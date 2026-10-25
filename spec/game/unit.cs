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
    /// 0x5972DB, 0x5971B6) select a unit with 0 and deselect one with 1
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

    /// bits 0..1 a phase (game seconds modulo 3), bit 5 while a live unit with +0x346 or a moving ambient object stands
    /// next to it: EEUnit::vf16 0x61648E checks it again every 3 s and returns 1 while bit 5 is set; the constructor
    /// 0x616BDD clears it
    [offset(0x44), found(c), guess]
    uint8 neighbour_flags;
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
    /// EEUCLand::vf45)
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
    [offset((ee)0x27C, (aoc)0x280), found(c), guess]
    int32 produce_left;

    /// room taken inside it: its occupants' transport sizes, at most its type's garrison_capacity
    [offset((ee)0x284, (aoc)0x288), found(c)]
    int32 garrisoned;

    /// beside garrisoned: entering (0x5E8830) adds the occupant type's slot 27 (0x6C, its transport size), 0x5E88C6
    /// sets it, 0x5EB0C4 zeroes both; only passed on, never read for its value (0x46CA7E, 0x5B8823 attribute 9)
    [offset((ee)0x288, (aoc)0x28C), found(c), guess]
    int32 garrison_load;

    /// a missile's climb left (ms)
    [offset((ee)0x28C, (aoc)0x290), found(c)]
    int32 flight_ms;

    /// its index in the movement group's slot list, set by the group dispatch 0x5DB641; init 1
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
    /// take rank / 5 x [0x917DC8] as their threshold when type +0x5D4 has bit 4
    [offset((ee)0x2DC, (aoc)0x2E0), found(c), guess]
    int32 group_rank;

    /// EEPMNaval::vf21's step test (0x550300 / AoC 0x55xxxx's read of +0x2E4) against TILE_SEA_CLASS
    [offset((ee)0x2E0, (aoc)0x2E4), found(c)]
    int32 sea_mode;

    /// ammunition left (the dump's Ammunition): its type's +0x5A0 on rearming
    [offset((ee)0x2E4, (aoc)0x2E8), found(c)]
    int32 ammo;

    /// the fire point of the next shot: fire 0x61E11C steps it (+1 modulo the fire points), 0x5FC749 looks up the
    /// projectile's start by it, EEE2DMuzzleFlash::vf12 reads it
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
    /// apart; till then contacts with it take the push branch
    [offset((ee)0x320, (aoc)0x324), found(c), guess]
    uint32 nudge_until;

    /// the out-of-synch dump's "Collision State" (0x5EB998): 0, 2 or 8 - at 2 the contact test 0x548A3C gives up (state
    /// 5000 / 0xFA6, move_mode 6) and resets it; about 12 movement model functions read it
    [offset((ee)0x330, (aoc)0x33C), found(c)]
    uint16 collision_state;

    /// init 1, set from its type's deletable (0x5B1F5B, 0x5B32CB) and cleared while frozen (0x5AF0BB): 0x5EC2C3 returns
    /// it and EEMKillUnit::vf10 0x4B7ED0 deletes only units it allows; the calamities' functors (fire, plague,
    /// hurricane) test it too
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

    /// set when its projectile hits a target of unit group 0x2A (0x61F6A4): with the stats' flag +0x15C it then attacks
    /// with animation 8 for 0 and 1.5 x its reload (EEUGAttackUnit::vf16, fire 0x61E19F), its splash skipping victims;
    /// a new goal clears it (assign_goal 0x5F05BD)
    [offset((ee)0x342, (aoc)0x34E), found(c), guess]
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

/// the game's functions unit.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class unit_functions
{
    [address((ee)0x0062CBB3, (aoc)0x00641A6A), cdecl]
    static extern char8 ballistic_game(EEUnit* p1, ee_unit_stats* p2, float32* p3, float32* p4, float32* p5);

    /// cdecl(unit, player index, new type data)
    [address((ee)0x0062F192, (aoc)0x00644E95), cdecl]
    static extern int32 unit_convert(EEUnit* u, int32 pi, EEUnitClass* new_td);

    /// cdecl(unit, other)
    [address((ee)0x0062D5A4, (aoc)0x00642489), cdecl]
    static extern float32 unit_distance(EEUnit* a, EEUnit* b);

}
