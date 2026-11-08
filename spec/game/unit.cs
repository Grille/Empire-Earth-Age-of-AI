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

    /// the next unit of its owner's list (EEPlayer +0x450): 0x628D0A inserts, 0x628E06 unlinks, 0x628CDD clears;
    /// population and set_diplomacy walk it
    [offset(0xC), found(c)]
    EEUnit* next;

    /// the previous unit of that list (the same three functions)
    [offset(0x10), found(c)]
    EEUnit* prev;

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

    /// the scenario editor's selection: its select 0x58E1E7 sets 1, its mouse and lasso actions (EESEAMouseClick::vf1
    /// 0x5972DB, 0x5971B6) select a unit with 0 and deselect one with 1 (the AoC decompilation: mIsEditorSelected)
    [offset(0x29), found(c), guess]
    uint8 selected;

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
    /// vf24: off the map - die 1 dies (its death shown), 0 vanishes: the scenario's Kill / Remove (0x5AF26F: (die, 0,
    /// 0), then the owner's drop when its list_node is cleared), the AI's deletions (1, 0, 0), unit_give's old one
    /// (0, 0, 1)
    [slot(24)]
    extern void destroy(int32 die, int32 p3, int32 p4);

    /// vf22, a building under construction (the AoC decompilation's IsFoundation) [guess]: the units' collection takes
    /// no animation frames from an object with it (0x502622: its record's frames 0) and draws a doppelganger of one
    /// on the map only for its owner
    [slot(22), guess]
    extern uint8 is_foundation();

    /// a path marker: its type 0x104 or 0xDF (the AoC decompilation's IsPathMarker) - the units' view test draws a
    /// remembered one only for its owner
    [address((ee)0x0046EFCF, (aoc)0x004708C9)]
    extern uint8 is_path_marker();

    /// what the units' view test asks a path point (type 0xDF) of its owner's: in both builds a shared `return false`
    /// (EEAreaEffect::vf8, the AoC decompilation's Fn27C0B4) - so none is drawn
    [address((ee)0x00651420, (aoc)0x0067C0B4)]
    extern uint8 path_point_shown();

    /// vf16 (the AoC decompilation's UpdateHidden; EEUnit's 0x61648E): hidden behind something - every 3 s checked
    /// again, else hidden_flags' bit 5. The units' opacity (EEROUnits.opacity) draws an obstacle or a moved unit see-through
    /// by it when "show hidden units" is 1
    [slot(16)]
    extern uint8 update_hidden();

    /// vf21: its graphics (the AoC decompilation's GetModel) - the units' opacity takes the model of its animation
    /// (EEGraphicObject.anim_model)
    [slot(21)]
    extern EEGraphicObject* graphics();

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

    /// thiscall(unit, slot): the slot's goal queue (unit +0x94 a vector of 0x28-byte slots; +4 a deque of goal
    /// pointers: +4 block map, +8 its size, +0xC the front's index, +0x10 the count; 0x400 a block) - slot 0 the
    /// orders, the front the current, shift appends
    [address((ee)0x005F3473, (aoc)0x005FA1CD)]
    extern void* goal_queue(int32 slot);

    /// bits 0..1 the hidden check's phase (game seconds modulo 3), bit 4 moved (MoveTo sets it, and the "show hidden
    /// units" toggle EEUIEMGToggleShowHiddenUnits::vf1 0x64A102 on the units with +0x346), bit 5 hidden: the hidden
    /// check EEComplexUnit::vf16 0x5EA87E sets it and the phase on the units on a moved unit's tile; EEUnit::vf16
    /// 0x61648E checks it again every 3 s and returns 1 while bit 5 is set; the constructor 0x616BDD clears it (the AoC
    /// decompilation: mHiddenFlags, kHiddenFlagMoved 0x10 / kHiddenFlagHidden 0x20, UpdateHidden)
    [offset(0x44), found(c)]
    uint8 hidden_flags;
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

    /// the units aboard or garrisoned (its allocator byte +0x6C, built by the constructor 0x5E8394):
    /// EEUnit::count_inside 0x42E347 returns its size, 0x483D97 skips an unload while it is empty
    [offset(0x70), found(c)]
    vector inside;

    /// the head of its contact list (nodes {unit, prev, next}): 0x5E8B77 adds, 0x5EF0AD removes, 0x5F23E9 clears it
    /// with contacts_count; the constructor 0x5E8394 zeroes both
    [offset(0xC4), found(c)]
    void* contacts;

    /// the count of contacts (0x5F240E adds one): EEPhysicsModel.collide (vf18 0x549E61, AoC vf19) runs only
    /// while it is not 0 or a blocked time is left
    [offset(0xC8), found(c)]
    int32 contacts_count;

    /// the scenario's unit attributes unit_variable1..3 (SM_UA 17..19) as ints (allocator byte +0xCC; init 0x5ECD26
    /// sizes it to 3): 0x5B8823 adds to, takes from or sets begin[attr - 17]
    [offset(0xD0), found(c)]
    vector unit_vars;

    /// the model it is drawn with now: the unit is a GEPhysicalModel from +0x130 (its matrix first), whose SetModel
    /// (an import of the engine's) keeps the model at its +0x40 - the animation's (start_anim: the type's +0x234[anim])
    [offset(0x170), found(c)]
    GEModel* model;

    /// its height: its stats' height x scale (airplanes recompute it each tick; the scenario's attribute 0xD adds to
    /// it)
    [offset(0x174), found(c)]
    float32 height;

    /// the current frame of its animation (the dump's Current Frame)
    [offset(0x178), found(c)]
    int32 anim_frame;

    /// radians/s, signed
    /// the turn rate now (radians/s, signed) - EEPhysicsModel.collide in both builds, model_turn_decide's caller
    [offset(0x17C), found(c)]
    float32 turn_rate;

    /// pitch velocity (radians/s): the movement models set it (an airplane's climb) or zero it
    [offset(0x180), found(c)]
    float32 pitch_rate;

    /// roll velocity (radians/s), 0 in every movement model
    [offset(0x184), found(c)]
    float32 roll_rate;

    /// its heading (radians) as the models give it to SetOrientationYPR
    [offset(0x188), found(c)]
    float32 yaw;

    /// its pitch (radians), SetOrientationYPR's second
    [offset(0x18C), found(c)]
    float32 pitch;

    /// its roll (radians), SetOrientationYPR's third
    [offset(0x190), found(c)]
    float32 roll;

    /// the tile of its collision (the dump's Collision Tile; -10000 none)
    [offset(0x1A4), found(c)]
    ivec2 collision_tile;

    /// a projectile's: the attack's type data
    [offset(0x1C0), found(c)]
    EEUnitClass* attacker_type;

    /// its active movement group: while it has one, max_speed is the group's
    [offset(0x1C8), found(c)]
    EEGroup* group;

    [offset(0x1CC), found(c)]
    EEUnit* chain;

    /// the unit whose chain (+0x1CC) this one is, referenced
    [offset(0x1D0), found(c)]
    EEComplexUnit* chained;

    /// an airplane's airport (its EEAirTrafficControl: +0x26C the state, +0x1C/+0x20 the tile), NULL none
    [offset(0x1D4), found(c)]
    void* air_traffic;

    /// set while inside a transport or building (EEUCTransport::vf63: none may board)
    [offset(0x1D8), found(c)]
    void* holder;

    /// the last unit attacking it, referenced; dropped once that unit's attack goal no longer targets it (0x5F1214)
    [offset(0x1DC), found(c)]
    EEComplexUnit* attacker;

    /// its script state (AISTATE)
    [offset(0x1E8), found(c)]
    EEUnitAction* ai_state;

    /// its goal (GOAL)
    [offset(0x1EC), found(c)]
    EEUnitGoal* goal;

    /// the goal object (0x5ED24B casts it)
    [offset(0x1F0), found(c)]
    EEUnitGoal* goal_object;

    /// an animal's herd (EEHerd)
    [offset(0x200), found(c)]
    void* herd;

    /// its behaviour (behaviour_table())
    [offset(0x208), found(c)]
    void* behaviour;

    /// a unit it is stuck against, referenced (0x5EFF37 releases the old one): the contact test (EEPhysicsModel::vf16
    /// 0x548A3C, EEPMNaval::vf16) sets it while its collision_state is 0 and the other stands, and clears it when the
    /// contact resolves; the re-path EEUnitClass::vf53 0x6225E7 avoids its tiles
    [offset(0x20C), found(c)]
    EEComplexUnit* blocker;

    /// init 2 pi or a random 0..2 pi; helicopters (EEPMHelicopter::vf2 0x54C45C) and ships (EEPMNaval::vf2 0x54ECF5)
    /// advance it each step and wrap it at 2 pi - cos(it) x type +0x48C their bobbing, type +0x490 a ship's rocking;
    /// airplanes keep their bank angle here (EEPMAirplane::vf23 0x55108F)
    [offset(0x220), found(c), guess]
    float32 bob_phase;

    /// seconds to its next heal search: state 16000 (EEUnitClass::vf29 0x61AD41) looks for a unit in sight to heal
    /// (EEUGHealUnit) and resets it to type +0x4A4; 0x600822 counts it down by the step (EEUCBasicPriest::vf45,
    /// EEUCLand::vf45) (the AoC decompilation: mHealSearchTimer)
    [offset(0x22C), found(c), guess]
    float32 heal_search_s;

    /// the push velocity (tiles/s)
    /// the push velocity {x, y} (tiles/s; the collision test adds, movement.slot_pm_push moves the unit by it x a step x
    /// game.phys_substeps and zeroes it) - EEPhysicsModel.collide in both builds
    [offset(0x230), found(c)]
    vec2 push;

    /// ms in an unresolved collision (the dump's Coll Resolut Cnt): + a step's ms each pushed step, 0 when it resolves
    [offset(0x248), found(c)]
    float32 blocked_ms;

    /// the tile setter 0x616F9D copies blocked_ms here and zeroes blocked_ms; on the next tile change with blocked_ms 0
    /// the model step (EEPhysicsModel::vf3 0x549A08) puts it back + step_ms, so the blocked time carries over tiles
    /// (AoC's new field is +0x250, this one stays)
    [offset(0x24C), found(c)]
    float32 blocked_ms_prev;

    /// game time (EEPhysics +0xD0): 0x5F0F7E asks its AI state (goalset_ask) while it isn't idle or the time is not
    /// past this, and while idle sets it to now + 350; entering idle (vf29 state 5000) sets -1
    /// (the AoC decompilation: mNextTriggerTime)
    [offset((ee)0x254, (aoc)0x258), found(c), guess]
    uint32 idle_ask_until;

    /// its animation's sound (the animation setter 0x5EFD6D)
    [offset((ee)0x258, (aoc)0x25C), found(c)]
    SESMInstanceNode* anim_sound;

    /// what a building makes now
    [offset((ee)0x25C, (aoc)0x260), found(c)]
    int32 producing;

    /// its animation (7/8 idle, 0xD walk, 2 carry ...: the setter 0x5EFD6D; the model from type +0x234[anim])
    [offset((ee)0x260, (aoc)0x264), found(c)]
    int32 anim;

    /// the movement mode
    /// the movement mode (0x549F82 picks the model's step by it; 2 no collisions) - the push (movement.slot_pm_push) sets an
    /// idle unit's pushed walk (+0x260 13) only while it is neither 0 nor 5, in both builds
    [offset((ee)0x264, (aoc)0x268), found(c)]
    int32 move_mode;

    /// the dump's CurrentPhysicsOrder (an airplane: 5 at a stop)
    [offset((ee)0x268, (aoc)0x26C), found(c)]
    int32 physics_order;

    /// the dump's Crew Hit Points
    [offset((ee)0x278, (aoc)0x27C), found(c)]
    int32 crew_hp;

    /// a building's work left on its production: the start 0x5EF9FF sets the item's time x rate (1 with the player's
    /// instant_build), 0x5EF6A7 zeroes it when done, the progress bar 0x64297D draws the total minus it
    /// (the AoC decompilation: mProductionTime)
    [offset((ee)0x27C, (aoc)0x280), found(c), guess]
    int32 produce_left;

    /// room taken inside it: its occupants' transport sizes, at most its type's garrison_capacity
    [offset((ee)0x284, (aoc)0x288), found(c)]
    int32 garrisoned;

    /// beside garrisoned: entering (0x5E8830) adds the occupant type's slot 27 (0x6C, its transport size), 0x5E88C6
    /// sets it, 0x5EB0C4 zeroes both; only passed on, never read for its value (0x46CA7E, 0x5B8823 attribute 9)
    /// (the AoC decompilation: mGarrisonPopulation)
    [offset((ee)0x288, (aoc)0x28C), found(c), guess]
    int32 garrison_load;

    /// a missile's climb left (ms)
    [offset((ee)0x28C, (aoc)0x290), found(c)]
    int32 flight_ms;

    /// its index in the movement group's slot list, set by the group dispatch 0x5DB641; init 1
    /// (the AoC decompilation: mFormationSlot)
    [offset((ee)0x298, (aoc)0x29C), found(c), guess]
    int32 group_index;

    /// its hit points at the start of its tick (a script condition compares them to see a hit)
    [offset((ee)0x29C, (aoc)0x2A0), found(c)]
    int32 hp_last;

    /// its own reload (ms, 0: its class's)
    [offset((ee)0x2A0, (aoc)0x2A4), found(c)]
    int32 reload;

    /// a transport's room left (EEUGLoadTransport::vf34; full below 1)
    /// 2 votes, UNIT_RELOAD's shift
    [offset((ee)0x2A4, (aoc)0x2A8), found(c)]
    int32 room;

    /// the out-of-synch dump's "Rotting Away Time" (0x5EBA48): set to its type's rot time (+0x548) when it dies (state
    /// 0x1772, 0x607B11); the scenario attribute 0xF changes it
    [offset((ee)0x2A8, (aoc)0x2AC), found(c)]
    int32 rot_ms;

    /// its own opacity (saved; the AoC decompilation's mOpacity): a cloakable unit's - the units' opacity
    /// (EEROUnits.opacity) draws it so, see-through below 1, and to an enemy dimmed to at most cloaked_opacity
    [offset(0x218), found(c)]
    float32 opacity;

    /// it can cloak (the AoC decompilation's mIsCloakable): the units' opacity draws it by its opacity, else at 1
    [offset((ee)0x339, (aoc)0x345), found(c)]
    uint8 cloakable;

    /// THE FILL'S (EEROUnits' FillUnit, collect.c uc_fill): its record gets these. Drawn in a tint of its own (the
    /// record's tinted; the tint the three floats at +0x1AC) [guess: what sets it]
    [offset((ee)0x33D, (aoc)0x349), found(c), guess]
    uint8 tint_on;

    [offset(0x1AC), found(c), guess]
    /// the tint: the fill copies the three floats into the record's tint while tint_on
    vec3 tint;

    /// a gate's opening (the record's gate_open)
    [offset(0x228), found(c)]
    float32 gate_open;

    /// how far between anim_frame and next_frame (the AoC decompilation's physical model's fraction)
    [offset(0x23C), found(c)]
    float32 fraction;

    /// a float per part of its graphics (EEGraphicObject's +4 vector) [guess]
    [offset(0x204), found(c), guess]
    float32* part_values;

    /// two lists of pointers and their counts (the fill puts each element's +0xC and +4 into its record) [guess]
    [offset(0x1B8), found(c), guess]
    void** list_a;

    [offset((ee)0x2EC, (aoc)0x2F0), found(c), guess]
    /// list_a's count (the fill's loop)
    uint32 list_a_n;

    [offset(0x1BC), found(c), guess]
    /// the second list (the fill puts each element's +0xC and +4 into the record's list_b_c / list_b_4)
    void** list_b;

    [offset((ee)0x2F0, (aoc)0x2F4), found(c), guess]
    /// list_b's count (the fill's loop)
    uint32 list_b_n;

    /// a vector of its goals' handlers (the first at +0x100, the end +0x104; each one's +0x10 -> +0x20 its kind): the fill
    /// asks the first of kind 5, 6 or 2 its vf6 [guess]
    [offset(0x100), found(c), guess]
    void** goals_first;

    [offset(0x104), found(c), guess]
    /// goals_first's end
    void** goals_end;

    /// AoC's: it is cloaked now (mIsCloaked) - AoC's units' opacity (0x63CEE2) draws a unit already see-through
    /// (show hidden units) by its cloak only while cloakable and this are set
    [offset((ee)none, (aoc)0x346), found(c)]
    uint8 cloaked;

    /// ms until it next looks for a target: counted down by a step's ms, reset to type +0x564 after a search; an
    /// attack-move zeroes it
    [offset((ee)0x2AC, (aoc)0x2B0), found(c)]
    int32 scan_ms;

    /// a priest's or prophet's power: Cast Calamity (EEMCastCalamity::vf10 0x4B455D) casts only when the calamity's
    /// cost is within it; EEUCBasicPriest::vf45 regenerates it, the scenario attribute 0xE sets it in percent of stats
    /// +0x148, the status box shows it
    [offset((ee)0x2D0, (aoc)0x2D4), found(c)]
    int32 power;

    /// set while it moves
    /// set while it moves (EEPhysicsModel.collide's first test in both builds)
    [offset((ee)0x2D8, (aoc)0x2DC), found(c)]
    int32 moving;

    /// the group dispatch 0x5DB641 sets round(group_index / members x 4), 0..4; the group's auto waypoints (0x5F0844)
    /// take rank / 5 x [0x917DC8] as their threshold when type +0x5D4 has bit 4 (the AoC decompilation: mFormationRank)
    [offset((ee)0x2DC, (aoc)0x2E0), found(c), guess]
    int32 group_rank;

    /// EEPMNaval::vf21's step test (0x550300 / AoC 0x55xxxx's read of +0x2E4) against TILE_SEA_CLASS
    [offset((ee)0x2E0, (aoc)0x2E4), found(c)]
    int32 sea_mode;

    /// ammunition left (the dump's Ammunition): its type's +0x5A0 on rearming
    [offset((ee)0x2E4, (aoc)0x2E8), found(c)]
    int32 ammo;

    /// the fire point of the next shot: fire 0x61E11C steps it (+1 modulo the fire points), 0x5FC749 looks up the
    /// projectile's start by it, EEE2DMuzzleFlash::vf12 reads it (the AoC decompilation: mWeaponTagPoint)
    [offset((ee)0x2E8, (aoc)0x2EC), found(c), guess]
    int32 muzzle;

    /// ms into the current frame (or the hold): + a step's ms each tick, 0 at a new animation
    [offset((ee)0x2FC, (aoc)0x300), found(c)]
    int32 anim_ms;

    /// the dump's Times Updated
    [offset((ee)0x304, (aoc)0x308), found(c)]
    int32 times_updated;

    /// the animation's next frame (the setter 0x5EFD6D)
    [offset((ee)0x308, (aoc)0x30C), found(c)]
    int32 next_frame;

    /// game time: the ship contact test EEPMNaval::vf16 0x54F84C sets now + 5000 when both ships stand and pushes them
    /// apart; till then contacts with it take the push branch (the AoC decompilation: mPushImmuneUntil)
    [offset((ee)0x320, (aoc)0x324), found(c), guess]
    uint32 nudge_until;

    /// the out-of-synch dump's "Collision State" (0x5EB998): 0, 2 or 8 - at 2 the contact test 0x548A3C gives up (state
    /// 5000 / 0xFA6, move_mode 6) and resets it; about 12 movement model functions read it
    [offset((ee)0x330, (aoc)0x33C), found(c)]
    uint16 collision_state;

    /// init 1, set from its type's deletable (0x5B1F5B, 0x5B32CB) and cleared while frozen (0x5AF0BB): 0x5EC2C3 returns
    /// it and EEMKillUnit::vf10 0x4B7ED0 deletes only units it allows; the calamities' functors (fire, plague,
    /// hurricane) test it too (the AoC decompilation: mIsDeletable)
    [offset((ee)0x334, (aoc)0x340), found(c), guess]
    uint8 deletable;

    /// init 1, the freeze effect 0x5AF0BB sets it to !frozen: the group orders (0x5DD463) skip members without it,
    /// transports refuse them (EEUCTransport::vf63)
    [offset((ee)0x335, (aoc)0x341), found(c), guess]
    uint8 commandable;

    /// an airplane's landing or takeoff clearance granted (the dump's Clearance granted)
    [offset((ee)0x336, (aoc)0x342), found(c)]
    uint8 clearance;

    /// the animation setter's third: the tick holds the frame until anim_ms passes type +0x318[anim]
    [offset((ee)0x337, (aoc)0x343), found(c)]
    uint8 anim_hold;

    /// frozen (the scenario effect)
    [offset((ee)0x33B, (aoc)0x347), found(c)]
    uint8 frozen;

    /// a gate locked: its own and allied units no longer pass (lets_pass)
    [offset((ee)0x33C, (aoc)0x348), found(c)]
    uint8 gate_locked;

    /// init 1, set when a path search for it finishes (0x4A24E1, 0x4A2B29, 0x4A32C0): the move state 8000's test
    /// (EEUnitClass::vf39 0x61BE4B) and EECPASGLand::vf127 wait for it; no write of 0 outside save and load
    /// (the AoC decompilation: mIsPathDone)
    [offset((ee)0x33E, (aoc)0x34A), found(c), guess]
    uint8 path_done;

    /// set each physics tick by its type's arrival test (slot 34, the distance to the waypoint), cleared when it stops
    [offset((ee)0x33F, (aoc)0x34B), found(c)]
    uint8 arrived;

    /// on a building's +0x30: built
    [offset((ee)0x340, (aoc)0x34C), found(c)]
    uint8 finished;

    [offset((ee)0x341, (aoc)0x34D), found(c)]
    uint8 ai_held;

    /// set when its projectile hits a target of unit group 0x2A (0x61F6A4) - a wall in the way: with the stats' flag
    /// +0x15C it then attacks with animation 8 for 0 and 1.5 x its reload (EEUGAttackUnit::vf16, fire 0x61E19F), its
    /// splash skipping victims, and its shots take the high arc (unit_functions.ballistic_game 0x62CBB3: stats +0x15C,
    /// this and animation 8); a new goal clears it (assign_goal 0x5F05BD), nothing else - the player 2026-11-07: "they
    /// first try a shot in a flat arc, if that hits a wall they switch to a high arc for their next shots; they don't
    /// switch back, even if the obstacle is cleared". tests/probes/wall_archers.eeai: longbows over a wall 272 damage in
    /// 2 minutes, in the open 414 - 0.66, the 1.5 x reload
    [offset((ee)0x342, (aoc)0x34E), found(c)]
    uint8 alt_attack;

    /// a building that is only a foundation yet: vf32 0x5F34EB sets it, vf22 0x5F3487 returns it; vf25 0x5ED737 shows
    /// the foundation's model by it (0x62C5C3), vf21 returns the foundation's stats instead of its own
    [offset((ee)0x345, (aoc)0x351), found(c)]
    uint8 foundation;

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
    /// its goal slots, 0x28 bytes each (+4 a deque of goals, +0x14 their count; slot 0 the orders: unit_goal_queue)
    [offset(0x94), found(c)]
    vector goal_slots;

    /// where its shots start (the dump's ProjectileStart)
    [offset(0x10C), found(c)]
    vec3 projectile_start;

    /// the place it returns to (0x5FE8E6 sets it; state 0x7EA sends it back; the target search looks around it)
    [offset(0x118), found(c)]
    vec3 guard_pos;

    /// its matrix (GETransformation, 3 x 4 floats by row): [1] [5] [9] the local y axis (facing), [3] [7] [11] the
    /// position in tiles
    [offset(0x130), found(b)]
    fixed float32 matrix[12];
    /// the height it flies its waypoint at (the dump's Cruise Altitude), reset by its type's slot 9 on a new tile
    [offset(0x210), found(c)]
    float32 cruise_alt;

    /// the heading (radians) to hold at guard_pos; -10000 none
    [offset(0x214), found(c)]
    float32 guard_heading;

    /// the speed, at least the type's minimum
    [offset(0x21C), found(c)]
    float32 speed_min;
    /// tiles/s (0 standing)
    [offset(0x238), found(c)]
    float32 speed;
    /// its scale (1 normal): size = type +0x474 x scale, height = its stats' height x scale
    [offset(0x240), found(c)]
    float32 scale;

    /// the type's size (+0x474) x the unit's scale, in tiles - a diameter to the circle test (EEPhysicsModel::vf15), the
    /// line's length to the ships' (EEPMNaval::vf16) - EEPhysicsModel.collide in both builds
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
    /// 1 / its top speed (its stats' +0xD0; its group's +0xA0 with the group speed on)
    [address((ee)0x00626391, (aoc)0x006394B0)]
    // the ripple timer (0x624D47, a unit's step on screen): a ripple at each of its model's ripple points every
    // max - (max - min) x speed x this ms (the type's ripple graphic +0x390: +0x6C min, +0x70 max) - none left at
    // 108% of a frigate's top speed (300 / 4000 ms), one every frame above
    [call_site("ripple_speed", (ee)0x00624D84, (aoc)0x0063943C)]
    extern float32 inv_speed();
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
    /// built (done 1) or a building site (0), on it and the units chained to it (+0x1D0): the scenario's Create
    /// (done, 0, 1) and its unit attribute Construction Complete (done, 1, 1); for types 0x2BA and 0x34 it sends a
    /// message through the server
    [address((ee)0x005EFF65, (aoc)0x005FE230)]
    extern void set_built(char8 done, char8 p3, char8 p4);
    /// its animation (anim) from frame (-1: the animation's first, its type's +0x350[anim]) and, unless it moves
    /// (+0x345), the model with it (the type's +0x234[anim]: a wall piece's straight, diagonal, sloped piece or post);
    /// flag kept at +0x337, 0 plays the animation's start sound
    [address((ee)0x005EFD6D, (aoc)0x005FE02A)]
    extern void start_anim(int32 anim, int32 frame, char8 flag);
    /// the goal as its order (append: after the ones it has)
    [address((ee)0x005F05BD, (aoc)0x005FE863)]
    [call_site("repair_goal", (ee)0x0046DA88, (aoc)0x004706DF)]         // the game's repair orders, handed over
    extern void assign_goal(EEUnitGoal* goal, int8 append, int32 prio);
    /// the building to build next, alone: its goals dropped, idle (5000), the building first in its build list (+0x60's
    /// deque) - the player's placing (EEMBuildUnit::vf10 0x4B3D85) for each selected citizen, 0x5E8B19 with shift
    /// (appended); the citizen's script takes it up from there, no goal is given
    [address((ee)0x005E8A3B, (aoc)0x005F60EA)]
    extern void build_next(EEUnit* building);
    /// a building queues the tech (a unit's tech id): the tech tree's checks (the player's vf 0x80, the node queueable),
    /// a citizen only under the population limit (player +0xAE8), at the queue's end (now 0) or through the user-input
    /// message (now 1). The citizens' order (train_citizens 0x48EE84) calls it once per citizen with the producer it
    /// shares the order out to - evenly over all of them, wherever the work is (sites.c TOWN WORK)
    [address((ee)0x005E8908, (aoc)0x005F5F97)]
    [call_site("train_citizen_at", (ee)0x0048F043, (aoc)0x00493D5F)]
    [call_site("train_citizen_at", (ee)0x0048F07E, (aoc)0x00493D9A)]
    extern void enqueue(int32 tech, char8 now);
    /// its tile (+0x1C / +0x20) changes to x, y: its map cell looked up (map +0x1816D4 by y * size + x; none past the
    /// map's square), and with link set and x changed its place in the world's x-sorted unit list again (EEWorld.resort,
    /// whose walk runs off the list's end for an x past the map - the player's crash at 594.7 min, Empire
    /// Earth.exe+228D6F under a holder's unload 0x5F04AD); (-10000, -10000) takes it off the map (garrisoned, loaded)
    [address((ee)0x00616F9D, (aoc)0x00629AC3)]
    [call_site("set_tile", (ee)0x005974DF, (aoc)0x005A76B3)]
    [call_site("set_tile", (ee)0x0059763E, (aoc)0x005A7814)]
    [call_site("set_tile", (ee)0x00597727, (aoc)0x005A78FE)]
    [call_site("set_tile", (ee)0x005ED341, (aoc)0x005FB285)]
    /// the placing by x, y (0x5F0252); AoC's (0x5FE611) is the other two placings' too - by a vector (0x5F033A) and by a
    /// matrix (0x5F043B) call it there (0x5FE76E, 0x5FE7A8), so their own calls are EE's only
    [call_site("set_tile", (ee)0x005F02B2, (aoc)0x005FE671)]
    [call_site("set_tile", (ee)0x005F03AB, (aoc)none)]
    [call_site("set_tile", (ee)0x005F04AD, (aoc)none)]
    /// AoC's only: a unit made beside another and put on its tile (0x6340A7, after a check of the place 0x4C7486)
    [call_site("set_tile", (ee)none, (aoc)0x0063448D)]
    [call_site("set_tile", (ee)0x00616B81, (aoc)0x006296AA)]
    [call_site("set_tile", (ee)0x0061F0BA, (aoc)0x00632D94)]
    [call_site("set_tile", (ee)0x0063861E, (aoc)0x0064E5EA)]
    extern void set_tile(uint32 x, uint32 y, char8 link);

    /// its matrix moved to pos (float[3]; its type's vf1 corrects it first: the ground's height) and turned by its
    /// heading, leaned with the ground there by its type's pitches and banks - the map's place (0x5117E6: a unit
    /// spawned, a scenario's), the editor's moves, a land unit's step (0x549A08) call it; a building laid out in a
    /// game (place_foundation) never does, so it stood level (an AA gun's legs in the hill)
    [address((ee)0x005EE7D7, (aoc)0x005FC90C)]
    extern void stand(float32* pos);

    /// AoC's only: its reload until the next blow, reload x a random factor between the game data's +0x984 and
    /// +0x988 (AoC's dbworld.dat: 0.8 and 1.2; read once into 0x963160 / 0x96315C; with the 2017 update's 84-row
    /// table loaded - a loose file, game.file_path - they lie past its end: heap memory, a factor near 0) - the 2017
    /// update sets the reload itself. The attack ticks' (EEUGAttackGround 0x61D186, EEUGAttackUnit 0x61D9A9) and the
    /// fire's (0x631395, 0x6313C7)
    [address((ee)none, (aoc)0x005FEA22)]
    [call_site("reload_wait", (ee)none, (aoc)0x0061D186)]
    [call_site("reload_wait", (ee)none, (aoc)0x0061D9A9)]
    [call_site("reload_wait", (ee)none, (aoc)0x00631395)]
    [call_site("reload_wait", (ee)none, (aoc)0x006313C7)]
    extern void reload_wait(int32 reload);

    /// freed: its slot 12 (+0x30), its references 0, then back to the game's pool (0x5F20FB / AoC 0x60096B)
    [address((ee)0x005ED71E, (aoc)0x005FB7FB), overrides(typeof(RTSUnit))]
    [vtable_site("unit_release")]
    extern void release();
}

/// a building or tree as a player remembers it under the fog (RTTI EEDoppelganger; EEComplexDoppelganger too): a
/// tile's doppelgangers (ee_tile.doppelgangers) - the units' collection draws them (EEROUnits.fill_doppelganger)
/// - this class one of an object without a complex unit (its virtual functions answer from the object's type and
/// owner: no pose, no animation, finished), EEComplexDoppelganger one with (what it was when last seen). The
/// collection's fills of them (EEROUnits.fill_doppelganger, fill_doppelganger_object) ask only these functions.
[c_type("ee_doppelganger"), vtable((ee)0x00831530, (aoc)0x00846E7C)]
unsafe struct EEDoppelganger
{
    /// the object it shows (the collection reads its type, its list node and its physics part)
    [offset(0x8), found(c)]
    EEUnit* unit;

    /// the player who remembers it
    [offset(0xC), found(c)]
    EEPlayer* player;

    /// vf6: its animation (0 here) - its graphics' model of it is the one drawn
    [slot(6), address((ee)0x004D84A2, (aoc)0x005AD67E)]
    extern int32 anim();

    /// vf8: finished (1 here) - the fill draws one not as a building site (its finished building's graphics, its
    /// height scaled by hp() over its type's full hit points) [guess: the name; the fill's use of it]
    [slot(8), address((ee)0x004B129C, (aoc)0x00418AA9)]
    extern uint8 finished();

    /// vf9: its animation's frame (0 here)
    [slot(9), address((ee)0x004D84A2, (aoc)0x005AD67E)]
    extern int32 frame();

    /// vf10: its graphics - here its owner's stats of the object's type's (unit_stats.gfx)
    [slot(10), address((ee)0x005F3D15, (aoc)0x00601D97)]
    extern EEGraphicObject* graphics();

    /// vf11: dead (0 here) - the fill gives one no health bar's place (bar_x 0) [guess: the name; the fill's use of
    /// it, as FillUnit gives an object of no hit points none]
    [slot(11), address((ee)0x004B129F, (aoc)0x004AC9F3)]
    extern uint8 dead();

    /// vf12: whether it has a pose of its own (transform(); 0 here: the fill puts it at its tile's middle)
    [slot(12), address((ee)0x004B129F, (aoc)0x004AC9F3)]
    extern uint8 has_transform();

    /// vf13: its pose into *out (a GETransformation; nothing here)
    [slot(13), address((ee)0x006048AB, (aoc)0x00557269)]
    extern void transform(void* @out);

    /// vf14: its scale - here its type's unknown_42c (an x87 float in st0)
    [slot(14), address((ee)0x005F3D35, (aoc)0x00601DB7)]
    extern float32 scale();

    /// vf15: its hit points (-1 here)
    [slot(15), address((ee)0x005F3D42, (aoc)0x00601DC4)]
    extern int32 hp();
}

/// a doppelganger of an object with a complex unit: what it was when the player last saw it (its pose +0x10, its
/// graphics +0x50, animation +0x54, scale +0x58, hit points +0x5C's low 31 bits and finished its top bit, frame +0x68,
/// dead +0x6C - the virtual functions' reads)
[@base(typeof(EEDoppelganger)), c_type("ee_doppelganger"), vtable((ee)0x00825164, (aoc)0x00846D5C)]
unsafe struct EEComplexDoppelganger
{
    [address((ee)0x00406A76, (aoc)0x005F1F3F), overrides(typeof(EEDoppelganger))]
    extern int32 anim();

    [address((ee)0x00406A7D, (aoc)0x005F1F46), overrides(typeof(EEDoppelganger))]
    extern uint8 finished();

    [address((ee)0x00406A8E, (aoc)0x005F1F57), overrides(typeof(EEDoppelganger))]
    extern int32 frame();

    [address((ee)0x00406A92, (aoc)0x005F1F5B), overrides(typeof(EEDoppelganger))]
    extern EEGraphicObject* graphics();

    [address((ee)0x00406A96, (aoc)0x005F1F5F), overrides(typeof(EEDoppelganger))]
    extern uint8 dead();

    [address((ee)0x004B129C, (aoc)0x00418AA9), overrides(typeof(EEDoppelganger))]
    extern uint8 has_transform();

    [address((ee)0x00406A9A, (aoc)0x005F1F63), overrides(typeof(EEDoppelganger))]
    extern void transform(void* @out);

    [address((ee)0x00406AAB, (aoc)0x005F1F74), overrides(typeof(EEDoppelganger))]
    extern float32 scale();

    [address((ee)0x00406AAF, (aoc)0x005F1F78), overrides(typeof(EEDoppelganger))]
    extern int32 hp();
}

/// a placement ghost: the building the player is placing as it is drawn under the mouse, green or red (the AoC
/// decompilation's EEGhostUnit), no map object (its constructor calls neither RTSUnit's nor EEUnit's) - made by
/// EEGhostUnit::Create (ghost_unit.create, 0x5F4608: new 0x334, its constructor 0x5F3D4C(type id, player, flag); an
/// airport's three runways and a granary's farms with it) into the world's engine_display.ghost_units, moved by the
/// preview (ghost_unit.preview), drawn from the records EEROUnits.fill_ghosts makes of them. Create's callers: 0x562D07,
/// 0x64F72A (the build menu's), 0x64F9C3 and the scenario's Move Unit (EESEAMoveUnit::vf1). Its destructor 0x5F3DE5
/// tells 0x4CEEF4 (the player's index, the type id) unless +0x331 is set.
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

    // a placement ghost (the world's engine_display.ghost_units): what the ghosts' fill (EEROUnits.fill_ghosts) copies
    // into its record - the same offsets in both builds (names the AoC decompilation's)

    /// where it is drawn (a GETransformation)
    [offset(0x4), found(c)]
    fixed float32 transform[16];

    /// its footprint's corner heights, float[12][12]
    [offset(0x44), found(c)]
    fixed float32 heights[144];

    /// the tile it is at
    [offset(0x284), found(c)]
    int32 tile_x;

    [offset(0x288), found(c)]
    int32 tile_y;

    /// its animation's frame, its animation
    [offset(0x28C), found(c)]
    int32 frame;

    [offset(0x290), found(c)]
    int32 anim;

    /// its footprint's cells, uint8[12][12] (0: the cell's edges drawn)
    [offset(0x2A0), found(c)]
    fixed uint8 cells[144];

    /// whether it can be placed there (the preview's; drawn green, else red)
    [offset(0x330), found(c)]
    uint8 can_place;

    /// GetModel: its player's stats of its type's graphics
    [address((ee)0x00506208, (aoc)0x0050CB62)]
    extern EEGraphicObject* model();

    /// GetScale: its player's stats of its type's height (an x87 float in st0)
    [address((ee)0x00506227, (aoc)0x0050CB81)]
    extern float32 scale();

    /// its animation and that animation's first frame (its graphics' table, +0x350 in the 2017 update, +0x360 in AoC)
    /// - a wall's ghost gets its piece's so (0x6383E9)
    [address((ee)0x005F4DB7, (aoc)0x00604A97)]
    extern void start_anim(int32 anim);
}

/// the placement ghosts' functions (EEGhostUnit's statics in the AoC decompilation)
static unsafe class ghost_unit
{
    /// Create: the ghosts replaced by one of type for pl at loc (2 floats) turned by rotation (0 for a type that
    /// doesn't turn), an airport's three runways and a granary's farms (not when moving) with it; inside the ghosts'
    /// lock. Callers: the main game's build menu, the scenario editor's unit list and Move Unit
    [address((ee)0x005F4608, (aoc)0x00604278), cdecl]
    static extern void create(float32* loc, int32 type, EEPlayer* pl, float32 rotation, uint8 is_move);

    /// EEPreviewPlacement: the ghosts moved to loc (2 floats) - their tiles, footprints, heights, whether they can be
    /// placed - and engine_display.ghosts_changed set; the main game's and the editor's mouse moves call it
    [address((ee)0x005F4DDA, (aoc)0x00604ABA), cdecl]
    static extern void preview(float32* loc);
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

    /// whether u on the ground of a building of type for pl is in the way: 0 not (some types and kinds), 1 it can
    /// step aside (pl's or an ally's, a mover: its stats' speed above 0), 2 it is in the way - the build site's test
    /// (EEUGBuildRepair.site_valid: any 2 makes the site invalid)
    [address((ee)0x0062E5D2, (aoc)0x00643978), cdecl]
    static extern char8 site_blocker(EEUnit* u, EEPlayer* pl, int32 type, char8 p4);

    /// AoC's only: the animation u attacks with - 0, 8 (the stats' +0x164 and u +0x34E) or 0xB (some types with u
    /// +0x355); the 2017 update picks 0 or 8 in the attack's tick itself
    [address((ee)none, (aoc)0x0063C9D9), cdecl]
    static extern int32 attack_anim(EEUnit* u);
}

/// a vector of units the game's code hands on (a wall's new pieces: unit_functions.wall_joints) - the runtime
/// library's: its units from begin to end, 4 bytes each
unsafe struct ee_unit_vec
{
    /// 0x62E73B: its first (the count (end - begin) >> 2)
    [offset(0x4), found(c)]
    EEUnit** begin;

    [offset(0x8), found(c)]
    EEUnit** end;
}

/// a vector of tiles the wall's code hands on (x, y: two ints each) - the runtime library's
unsafe struct ee_tile_vec
{
    [offset(0x4), found(c)]
    int32* begin;

    [offset(0x8), found(c)]
    int32* end;
}

/// a unit's script state, its +0x1E8 (EEUnitAction: a state of its .tai, engine-notes.md "Unit AI scripts"; its RTTI
/// in a running game, every unit's)
[c_type("ee_ai_state")]
unsafe struct EEUnitAction
{
    /// its goal handlers: ask (0x619FF5, AoC 0x62CD8B) walks begin +0x8 to end +0xC, asking each (0x618CCB) until one
    /// gives the unit a goal
    [offset(0x8), found(c), guess]
    vector handlers;

    /// the state's id, read by every condition through a unit's +0x1E8
    [offset(0x24), found(c)]
    uint32 id;

    /// its goal handlers asked in turn (the vector +0x8 / +0xC, 0x618CCB each) until one gives the unit a goal: the
    /// unit's step asks while it isn't idle or its idle_ask_until is past (0x5F0F7E)
    [address((ee)0x00619FF5, (aoc)0x0062CD8B)]
    [call_site("goalset_ask", (ee)0x005F0FFA, (aoc)0x005FF364)]
    [call_site("goalset_ask", (ee)0x005F1029, (aoc)0x005FF393)]
    extern int32 ask(EEUnit* u, int32* @out);
}

/// the auto-target search's state: 0x62E99D's local, filled by 0x62EA64 (AoC's the same), that target_ok and the
/// scorer 0x6399CA run on
[c_type("ee_target_search")]
unsafe struct EETargetSearch
{
    /// the player of the unit searching: target_ok skips its own and those it isn't at war with
    [offset(0x20), found(c)]
    EEPlayer* player;
}

/// the game's functions unit.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class unit_functions
{
    /// the auto-target search's test of a candidate (thiscall on the searcher 0x62EA64 fills: +0x1C the unit
    /// searching, +0x20 its player): not its own player's, an enemy by the stance (player +0x9E4), alive (+0x3C), not
    /// +0x33B - then 0x6399CA scores it; the searches 0x63969A (2 calls) and 0x639D7D (2) of 0x62E99D, which a unit's
    /// idle scan 0x61AC11 runs
    [address((ee)0x00639993, (aoc)0x0064FFCA), thiscall]
    [call_site("target_ok", (ee)0x00639918, (aoc)0x0064FF4F)]
    [call_site("target_ok", (ee)0x00639F4A, (aoc)0x006505A4)]
    [call_site("target_ok", (ee)0x0063A072, (aoc)0x006506CC)]
    [call_site("target_ok", (ee)0x0063A145, (aoc)0x0065079F)]
    static extern char8 target_ok(EETargetSearch* searcher, EEUnit* u);

    /// the auto-target search a unit's idle scan runs (0x61AC11, the scripts' CanISeeEnemy): the best enemy around u
    /// by the scorer 0x6399CA (its weights per weapon kind at 0x8CFA28: a preferred class at once, then attacking u, in
    /// range, slower, shorter range, the to-hit bonus, nearer) within its sight, NULL none - the target an attack
    /// goal's constructor takes
    [address((ee)0x0062EBD2, (aoc)0x00643F78), cdecl]
    static extern EEUnit* find_target(EEUnit* u);

    /// cdecl(unit, its attack goal, the new target): the goal's target changed in place (its vf27 +0x6C, the old one's
    /// reference released) and the unit sent to CheckRange (2013) as after a new order - the goals under it (an
    /// attack-move) stay
    [address((ee)0x0062CD12, (aoc)0x00641BC9), cdecl]
    static extern void attack_retarget(EEUnit* u, EEUnitGoal* goal, EEUnit* target);

    [address((ee)0x0062CBB3, (aoc)0x00641A6A), cdecl]
    static extern char8 ballistic_game(EEUnit* p1, ee_unit_stats* p2, float32* p3, float32* p4, float32* p5);

    /// cdecl(shooter, target, &blocker, ?, ?): the line of fire from the shooter to its target - 1 when an object
    /// stands on it, *blocker that object (the target itself when nothing else does). The AI's target choice
    /// (EECPAttackSubGroupUnit::vf4 0x4620DF, AoC 0x463D50) asks it only for a unit that can't lob (unit_type
    /// high_arc 0, ee_unit_stats lob 0) and drops the target when the blocker is another object: a warship 4 tiles off
    /// a coast walled on its last land tile never attacked the houses behind it (tests/probes/wall_ships.eeai)
    [address((ee)0x00637F5A, (aoc)0x0064DF23), cdecl, guess]
    static extern char8 line_of_fire_game(EEUnit* shooter, EEUnit* target, EEUnit** blocker, uint32* p4, int32* p5);

    /// cdecl(x, y, target x, target y, &blocker, ?, ?): line_of_fire_game from a tile (the same choice's check of the
    /// spot it would shoot from)
    [address((ee)0x00637D65, (aoc)0x0064DD2E), cdecl, guess]
    static extern char8 line_of_fire_at_game(uint32 x, uint32 y, uint32 tx, uint32 ty, EEUnit** blocker, uint32* p6, uint32* p7);

    /// cdecl(unit, player index, new type data): the unit made anew as that type (0x62F2E5) with its share of hit
    /// points, the old one deleted - a line upgrade's (0x5BA59D), a wall piece's turn into a gate; no owner's change
    /// (unit_give)
    /// gate_convert: build_gate's (0x6022E3, AoC 0x61080D), each of its five pieces in turn; upgrade_convert: the line
    /// upgrade's, each unit of the old type (0x5BA59D, AoC 0x5CACB4)
    [address((ee)0x0062F192, (aoc)0x00644E95), cdecl]
    [call_site("gate_convert", (ee)0x00602456, (aoc)0x006109F2)]
    [call_site("upgrade_convert", (ee)0x005BA65A, (aoc)0x005CAE5D)]
    static extern int32 unit_convert(EEUnit* u, int32 pi, EEUnitClass* new_td);

    /// cdecl(piece, layout code, raised corners): a wall piece's model and turn by its place in the wall (0x63842B:
    /// codes 1-2 straight, anim 9, along y or x; 3-4 diagonal, 10; 5-8 the straight piece with a slope built in, 6, its
    /// uphill side by the turn; 9-12 the diagonal one, 3; else the round post, 11), then its matrix (0x5F043B). The
    /// layout (0x636CDC) picks a sloped piece only where the ground between two tiles rises 0.4 or more - which fits its
    /// mesh's one slope (0.42 over its length) and little ground; raised corners (0x517C10: how many of the tile's
    /// corners stand 0.4 above its lowest) goes unread. The wall's placing (0x629DDB: a new piece, a piece joined anew)
    /// and the editor's (0x66CB35, finished pieces) call it; the drag's preview has its own (0x6383E9)
    [address((ee)0x00638628, (aoc)0x0064E647), cdecl]
    [call_site("wall_piece", (ee)0x00629F9C, (aoc)0x0063ED73)]
    [call_site("wall_piece", (ee)0x0062A0F9, (aoc)0x0063EF01)]
    [call_site("wall_piece", (ee)0x0066CBF6, (aoc)0x00685CF3)]
    static extern void wall_piece(EEUnit* u, int32 code, int32 raised);

    /// cdecl(tiles, layout codes, the wall's tech, player, its turn): the ghosts replaced by one per tile (8 bytes each:
    /// x, y), each the piece of its layout code (0x6383E9: as wall_piece picks it), at the ground under its tile's
    /// middle, flat; the vectors are the runtime library's (begin +4, end +8) - inside the ghosts' lock. Its one caller
    /// is wall_preview
    [address((ee)0x005F4A1C, (aoc)0x006046FC), cdecl]
    [call_site("wall_ghosts", (ee)0x00629DC7, (aoc)0x0063EB9E)]
    static extern void wall_ghosts(ee_tile_vec* tiles, void* codes, uint32 tech, EEPlayer* pl, float32 rotation);

    /// cdecl(tiles, the wall's tech, player index): a dragged wall's preview - the tiles' repeats taken out, the rest
    /// laid out (0x636CDC) and made ghosts (wall_ghosts). The interface's mouse moves call it, the ghosts' lock held
    [address((ee)0x00629D75, (aoc)0x0063EB4C), cdecl]
    static extern void wall_preview(ee_tile_vec* tiles, uint32 tech, int32 pi);

    /// cdecl(from, to (2 floats each), the tiles): the line a wall is laid along, from tile to tile (0x63379D) - the
    /// wall's placing's first step (0x629DDB). Not the preview's: that samples the mouse's line a tile a step
    [address((ee)0x006357BF, (aoc)0x0064B747), cdecl]
    static extern void wall_line(float32* from, float32* to, ee_tile_vec* tiles);

    /// cdecl(from x, y, to x, y, the wall's type, player, cost it, p6, a vector of tiles): a wall laid - wall_line,
    /// wall_extend, wall_layout, a piece per tile (wall_piece), wall_joints. cost 0: finished at once, nothing paid. p6
    /// goes to the placement test; the vector is the caller's, unread here
    [address((ee)0x00629DDB, (aoc)0x0063EBB2), cdecl]
    static extern void wall_place(float32 x0, float32 y0, float32 x1, float32 y1, EEUnitClass* td, EEPlayer* pl, char8 cost, int32 p6, ee_tile_vec* unused);

    /// cdecl(the line's tiles, player, the wall's type, the tiles to lay): the line with each end carried on along it
    /// through pl's wall of that type it meets (0x635753), a single tile by what stands around it. Reads the map only
    [address((ee)0x00631D51, (aoc)0x00647BEF), cdecl]
    static extern int32 wall_extend(ee_tile_vec* line, EEPlayer* pl, EEUnitClass* td, ee_tile_vec* tiles);

    /// cdecl(tiles, layout codes, map): each tile's code - straight, diagonal, sloped, post (wall_piece's)
    [address((ee)0x00636CDC, (aoc)0x0064CCA5), cdecl]
    static extern void wall_layout(ee_tile_vec* tiles, void* codes, EEMap* map);

    /// cdecl(pieces, the layout's tiles, player, map): after a wall's layout (0x629DDB, the editor's 0x66CB35) each of its
    /// pieces with more than two of the player's built walls-family objects within a tile becomes a post (anim 11),
    /// turned back to yaw 0, its matrix made anew from its own (0x5F043B) - where walls meet or cross
    [address((ee)0x0062E73B, (aoc)0x00643AE1), cdecl]
    [call_site("wall_joints", (ee)0x0062A159, (aoc)0x0063EF61)]
    [call_site("wall_joints", (ee)0x0066CC55, (aoc)0x00685D52)]
    static extern void wall_joints(ee_unit_vec* pieces, void* tiles, EEPlayer* pl, EEMap* map);

    /// cdecl(unit, other)
    [address((ee)0x0062D5A4, (aoc)0x00642489), cdecl]
    static extern float32 unit_distance(EEUnit* a, EEUnit* b);

    /// cdecl(unit, player index, count): the scenario's Owner effect (0x5ACC11: count 0) - the unit made anew for the
    /// player (0x62F2E5: its type's create, its hit points and state carried over), the old one destroyed (0, 0, 1);
    /// count 1 adds a conversion to the statistics. The new unit (its +0x30); its id comes with the player's adopt
    [address((ee)0x00638670, (aoc)0x0064E7DA), cdecl]
    static extern EEUnit* unit_give(EEUnit* u, int32 pi, char8 count);

    /// cdecl(attribute, percent, unit, value, hi, outside, op): the scenario's Unit-Attribute effect on one unit -
    /// value a random one of value..hi when hi isn't 0 (outside: outside the range), op SM_INCREASE / SM_DECREASE /
    /// SM_EXACTLY, percent of the type's value; results not below 0 (docs/triggers.md "2. Attribute effects")
    [address((ee)0x005B8823, (aoc)0x005C8F28), cdecl]
    static extern void unit_attr_set(sm_unit_attr attr, char8 percent, EEUnit* u, int32 value, int32 hi, char8 outside, sm_unit_op op);

}
