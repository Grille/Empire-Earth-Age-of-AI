/// an attack group (EECPAttackGroup)
[c_type("ee_attack_group")]
unsafe struct EECPAttackGroup
{
    [offset(0x8), found(c)]
    EECPAttackSubGroup** subgroups_begin;

    /// (its task +0x28's +0x1C)
    [offset(0x44), found(b)]
    EEUnit* target_unit;

    /// the player it attacks
    [offset(0x48), found(b)]
    EEPlayer* target;
}

/// the AI's combat manager, player +0xB8C (EECPCombatManager)
[c_type("ee_combat_mgr")]
unsafe struct EECPCombatManager
{
    [offset(0x48), found(c)]
    ee_attack_cmd* commander;

    [offset(0x4C), found(b)]
    uint8* _0x4c;

    [offset(0x62), found(v)]
    uint8 att_62;

    /// thiscall(cm, task, strategy, unit ids)
    [address((ee)0x0047067A, (aoc)0x0047360A)]
    extern char8 attack_plan_ok(void* task, void* strategy, void* ids);

    [address((ee)0x0046FFB7, (aoc)0x00472EC3)]
    extern void revenge(EEPlayer* target);

    /// the unit (its state, unit +0x30) in none of the combat manager's groups (the game's caller 0x471F29 its own)
    [address((ee)0x004707D8, (aoc)0x0047377E)]
    extern char8 unit_free(void* unit_state);
}

/// the object at the combat manager's +0x4C
[c_type("void")]
unsafe struct ee_combat_mgr_4c
{
    [offset(0x48), found(s)]
    block _0x48;
}

/// the combat manager's commands, AM +0x48
unsafe struct ee_attack_cmd
{
    /// the groups' deque (its count +0x10)
    [offset(0x4), found(s)]
    deque groups;

    /// thiscall(attack manager, task, strategy, unit ids) -> the attack group
    [address((ee)0x0045106C, (aoc)0x00451D8E)]
    extern EECPAttackGroup* new_group(void* task, void* strategy, void* ids);

    [address((ee)0x00450E07, (aoc)0x00451A8A)]
    [call_site("unit_to_group", (ee)0x004824BF, (aoc)0x004869F3)]
    extern char8 unit_to_group(EEUnit* u);
}

/// an attack subgroup (EECPAttackSubGroup, EECPASG*)
[c_type("ee_attack_subgroup")]
unsafe struct EECPAttackSubGroup
{
    /// its members
    [offset(0x54), found(c)]
    EECPAttackSubGroupUnit** units_begin;

    /// the end of its member records' vector (units_begin)
    [offset(0x58), found(c)]
    EECPAttackSubGroupUnit** units_end;

    [offset(0x60), found(c)]
    deque route;

    [offset(0x70), found(c)]
    uint32 waypoints;

    /// its target: the game sets it with EECPILocation::set (0x47C8B7)
    [offset(0x100), found(c)]
    EECPILocation target_rec;

    [offset(0x104), found(c)]
    int32 goal_x;

    [offset(0x108), found(c)]
    int32 goal_y;

    [offset(0x118), found(c)]
    EEUnit* target_unit;

    /// the player it attacks
    [offset(0x11C), found(b)]
    EEPlayer* target;

    [offset(0x124), found(c)]
    int32 mission;

    /// its task's +0x2C (the task embedded at +0xFC; the combat manager sets 2 or 3 there, 0x46F289, 0x46F4FF,
    /// 0x47111D, 0x472427): route_done (0x44BDBF) drops up to 7 waypoints at once when it is 3 or the mission 10
    [offset(0x128), found(c), guess]
    int32 task_kind;

    /// its task's +0x30, set whenever a goal location is filled in (0x453E10, 0x4543BC): vf5 0x459B93 falls back to
    /// mission 8 at +0xE4 while it is 0; route_done clears it when no transport is found, sea_update 0x44ED11 switches
    /// missions only while it is set
    [offset(0x12C), found(c), guess]
    uint8 task_valid;

    /// the player index the path search runs for: the head of the movement record its members' types fill (their vf10)
    [offset((ee)0x160, (aoc)0x164), found(c)]
    int32 move_player;

    /// a byte per terrain kind: may all its members enter it (1, then ANDed by each member's type)
    [offset((ee)0x164, (aoc)0x168), found(c)]
    fixed uint8 terrain_ok[17];

    /// the attack group it belongs to
    [offset((ee)0x18C, (aoc)0x190), found(c)]
    EECPAttackGroup* group;

    /// its own
    [offset((ee)0x190, (aoc)0x194), found(c)]
    EEPlayer* player;

    /// the path request plan_route queued
    [offset((ee)0x194, (aoc)0x198), found(c)]
    int32 route_request;

    /// game time before which update only gathers (vf +0x1C0) instead of acting
    [offset((ee)0x1A4, (aoc)0x1A8), found(c)]
    uint32 hold_until;

    /// game time of its next update (the caller runs its slot 90 once it passes)
    [offset((ee)0x1A8, (aoc)0x1AC), found(c)]
    uint32 next_update;

    /// vf91 0x45CCC3 picks a kind (2/3/4/7/9) by random ratios when it splits the group, not while it is 10; vf37
    /// 0x45BFC9 chooses by it which units leave (4 a random 30..70 %, 11 every free unit); update's state 0x3E9 sets 10
    /// (0x4491DD), message 0x232A 11
    [offset((ee)0x1CC, (aoc)0x1D0), found(c), guess]
    int32 split_kind;

    [offset((ee)0x1D0, (aoc)0x1D4), found(c)]
    int32 state;

    /// the route's last land waypoint before it leaves land, where the units board
    [offset((ee)0x224, (aoc)0x228), found(c)]
    ivec2 embark;

    /// the route's point where it comes ashore
    [offset((ee)0x22C, (aoc)0x230), found(c)]
    ivec2 landing;

    /// where it is
    [offset((ee)0x234, (aoc)0x238), found(c)]
    int32 x;

    [offset((ee)0x238, (aoc)0x23C), found(c)]
    int32 y;

    [address((ee)0x00442AD8, (aoc)0x00442220)]
    extern char8 air_give_goal(EEUnit* u, EEUnitGoal* goal, int32 a, int32 prio);

    /// EECPASGAir slot 90: update(now)
    [address((ee)0x00442DBA, (aoc)0x0045019D)]
    extern void air_update(int32 now);

    [address((ee)0x0045CBEA, (aoc)0x0045E7F9)]
    extern char8 give_goal(EEUnit* u, EEUnitGoal* goal, int32 a, int32 prio);

    [address((ee)0x004472A9, (aoc)0x004466DD)]
    extern void land_stats();

    /// its slot 97: route arrived, waypoints in the deque at +0x60
    [address((ee)0x0044BDBF, (aoc)0x0044B53D)]
    extern void route_done();

    /// EECPASGSeaTransport slot 90: update(now)
    [address((ee)0x0044FB92, (aoc)0x0044E9BB)]
    extern void sea_transport_update(int32 now);

    /// EECPASGSea slot 90: update(now)
    [address((ee)0x0044ED11, (aoc)0x0044F70E)]
    extern void sea_update(int32 now);

    [address((ee)0x0045AD2F, (aoc)0x0045C925)]
    extern void stats();

    /// EECPASGLand slot 90: update(now)
    [address((ee)0x004491DD, (aoc)0x004488CE)]
    extern void update(int32 now);

    /// its slot 94: void thiscall(int start[2], int dest[2])
    [address((ee)0x0044BC57, (aoc)0x0044B3C7)]
    extern void plan_route(int32* start, int32* dest);
}

/// an attack subgroup's unit record (EECPAttackSubGroupUnit, EECPASGU*)
[c_type("ee_asg_unit")]
unsafe struct EECPAttackSubGroupUnit
{
    [offset(0x44), found(c)]
    EEUnit* unit;

    /// set while it fights
    [offset(0x48), found(c)]
    void* engaged;

    [offset(0x4C), found(c)]
    EECPAttackSubGroup* subgroup;

    /// float thiscall(info *)
    [address((ee)0x00461854, (aoc)0x00463494)]
    extern float32 score(ee_asgu_info* info);
}

/// what asg_unit_score scores
unsafe struct ee_asgu_info
{
    [offset(0x4), found(s)]
    EEUnit* enemy;
}

/// a route planner's node
unsafe struct ee_route_node
{
    [offset(0x3C), found(s)]
    int32 x;

    [offset(0x40), found(s)]
    int32 y;
}

/// the land attack route planner (EEAIGPGModifierLandAttackSubGroupRoutePlanner)
[c_type("ee_route_planner")]
unsafe struct EEAIGPGModifierLandAttackSubGroupRoutePlanner
{
    /// zeroed by the base constructor 0x413FE1, the path search 0x4A1681 hands it over: cost 0x4141E8 compares its
    /// +0x50's +0x50 (an area id) for node kind 0xD, vf2 0x41459A reads a terrain byte at its +0x5C + kind - the
    /// request's terrain mask?
    [offset(0x8), found(c), guess]
    void* search;

    /// cost factors by node kind (1 unchanged), from the subgroup's slot 21 in plan_route: [0] kind 4, [1] kinds 0-2
    /// and 5-11, [2] kind 0xE, [3] kind 0xD, [4] the danger, [5] kind 3
    [offset(0xC), found(c)]
    fixed float32 weights[15];

    [offset(0x48), found(c)]
    int32 player_index;

    /// the target player's index (-1 none)
    [offset(0x4C), found(c)]
    int32 target_index;

    /// the subgroup it plans for
    [offset(0x50), found(c)]
    EECPAttackSubGroup* subgroup;

    /// the area id of the route's start
    [offset(0x54), found(c)]
    int16 start_area;

    /// the area id of the route's end
    [offset(0x56), found(c)]
    int16 dest_area;

    /// its slot 1: int thiscall(edge*, node*)
    [address((ee)0x004141E8, (aoc)0x00410074)]
    extern int32 cost(int32* edge, ee_route_node* node);

    /// plan_route's weights[5] == 0 (the constructor 0x414187): while 0, cost multiplies a node in an area other than
    /// the start's and the goal's by 50
    [offset(0x58), found(c), guess]
    uint8 free_crossing;

    /// always 1 from the constructor: vf2 then returns 0 for a kind 0xD node of its own or an allied player's object
    /// (the player's +0x9E4 table)
    [offset(0x59), found(c), guess]
    uint8 owner_test;
}

/// an attack strategy (EECPCombatStrategy)
[c_type("ee_strategy"), vtable((ee)0x0082795C, (aoc)0x0083C0A4)]
unsafe struct EECPCombatStrategy
{
    /// 0x473CE2 scales a shortfall of its argument (+0x78) by it to share units into count_0..2 (AoC has a fourth
    /// weight after weight_2)
    [offset(0x14), found(c), guess]
    float32 weight_0;

    /// the same for the argument's +0x74 (0x473CE2)
    [offset(0x18), found(c), guess]
    float32 weight_1;

    /// the same for the argument's +0x7C (0x473CE2)
    [offset(0x1C), found(c), guess]
    float32 weight_2;

    /// 0x4753A9 sets it from an object's +0x74 before new_group; init, reset and the copies (0x473C2E, 0x473E71) handle
    /// it
    [offset((ee)0x20, (aoc)0x24), found(c), guess]
    int32 target_ref;

    /// 0x4753A9 sets it from the looked-up object's int16 +0x70 (an area id?)
    [offset((ee)0x24, (aoc)0x28), found(c), guess]
    int16 target_area;

    /// the total 0x473CE2 splits into count_0..2 (the rest into count_2); its callers set 1..4, 1..20 or a rounded
    /// count, always the same as units_max (0x47111D, 0x4554xx)
    [offset((ee)0x28, (aoc)0x2C), found(c), guess]
    int32 units;

    /// written with the same value as units by every caller; no reader of its own found
    [offset((ee)0x2C, (aoc)0x30), found(c), guess]
    int32 units_max;

    /// units x weight_0, rounded (0x473CE2)
    [offset((ee)0x30, (aoc)0x34), found(c), guess]
    int32 count_0;

    /// units x weight_1, rounded (0x473CE2)
    [offset((ee)0x34, (aoc)0x38), found(c), guess]
    int32 count_1;

    /// the rest of units (0x473CE2; AoC has a fourth count after it)
    [offset((ee)0x38, (aoc)0x3C), found(c), guess]
    int32 count_2;

    /// init sets 0 (AoC adds a weight and a count before it: its init 0x477084, reset 0x47717B and copy 0x4773BF have
    /// it at +0x44)
    [offset((ee)0x3C, (aoc)0x44), found(c)]
    int32 _0x3c;

    /// init sets 1
    [offset((ee)0x40, (aoc)0x48), found(c)]
    uint8 _0x40;

    [address((ee)0x00473BC4, (aoc)0x00477084)]
    extern void init();

    [address((ee)0x00473C99, (aoc)0x0047716D)]
    extern void free();

    [address((ee)0x00473CA7, (aoc)0x0047717B)]
    extern void reset();
}

/// an attack task (EECPAGTask)
[c_type("ee_task"), vtable((ee)0x008265FC, (aoc)0x0083A6C0)]
unsafe struct EECPAGTask
{
    /// where it heads (task_init constructs it there)
    [offset(0x4), found(c)]
    EECPILocation location;

    /// the unit it attacks, referenced (saved as its id)
    [offset(0x1C), found(c)]
    EEUnit* target_unit;

    [offset(0x20), found(c)]
    EEPlayer* target_player;

    /// saved through stream slot 0x44 (floats); the subgroup's vf5 copies a nonzero one to its route weight +0x19C
    /// (else vf82 0x45AF8C, random 0.01..1), which vf21 0x45B045 passes as the route's danger factor (weights[4])
    [offset(0x24), found(c), guess]
    float32 danger_weight;

    [offset(0x28), found(c)]
    int32 _0x28;

    [offset(0x2C), found(c)]
    int32 _0x2c;

    [offset(0x30), found(c)]
    uint8 _0x30;

    /// an attack task
    [address((ee)0x00441FB3, (aoc)0x0044162F)]
    extern void init();

    [address((ee)0x00442283, (aoc)0x004417F5)]
    extern void free();

    [address((ee)0x00442246, (aoc)0x00441912)]
    extern void set_target(EEUnit* t);
}

/// a location an attack heads for - a task's +0x4, a subgroup's target (EECPILocation): a tile, or a unit (its
/// position taken when the tile is none, 0x47C537); 0x18 bytes (set copies five fields; AoC the same)
[size(0x18), c_type("ee_location"), vtable((ee)0x008265F4, (aoc)0x0083A6B4)]
unsafe struct EECPILocation
{
    /// the tile (point_set at +4 sets it from int x, y)
    [offset(0x4), found(c)]
    ivec2 tile;

    /// the unit's id (0: a place)
    [offset(0xC), found(c)]
    int32 unit_id;

    /// 1: a unit (as the mod sets it); copied with the rest
    [offset(0x10), found(c)]
    int32 kind;

    [address((ee)0x0047C4E8, (aoc)0x00480873)]
    extern void init();

    [address((ee)0x0047C8B7, (aoc)0x00480C42)]
    extern void set(EECPILocation* from);
}

/// attack's data of the game, places in its code and numbers
static unsafe class attack
{
    /// the land unit records' vtables of the attack subgroups: base, hero, priests and prophets
    [address((ee)0x00827844, (aoc)0x0083BF4C)]
    [address((ee)0x008253C0, (aoc)0x0083936C)]
    [address((ee)0x00825708, (aoc)0x008396BC)]
    [address((ee)0x008257F0, (aoc)0x008397A8)]
    [address((ee)0x00825458, (aoc)0x00839404)]
    [address((ee)0x00825540, (aoc)0x008394F0)]
    static vftable asgu_vtables;

    /// the attack subgroups' vtables
    [address((ee)0x00827620, (aoc)0x0083BD20)]
    [address((ee)0x00826610, (aoc)0x0083A6D8)]
    [address((ee)0x00826818, (aoc)0x0083A8E0)]
    [address((ee)0x00826F10, (aoc)0x0083AFEC)]
    [address((ee)0x008270F8, (aoc)0x0083B1D8)]
    [address((ee)0x00827318, (aoc)0x0083B9F0)]
    static vftable asg_vtables;

    /// EECPASGAir vf85: the waiting units' pop (asg_queue.c)
    [address((ee)0x00442718, (aoc)0x00441E02)]
    static code call_asg_air_queue_pop;

    /// the other subgroups' vf85 (0x45BD56): the waiting units' pop
    [address((ee)0x0045BDA2, (aoc)0x0045D974)]
    static code call_asg_queue_pop;

    /// thiscall(queue): releases the front unit, removes it
    [address((ee)0x00482028, (aoc)0x0048655C)]
    static code fn_asg_queue_pop;

    /// attack.asgu_vtables
    [value(2)]
    static uint32 slot_asgu_score;

    /// the attack subgroups
    [value(57)]
    static uint32 slot_asg_give_goal;

    /// attack.vt_asg_land
    [value(94)]
    static uint32 slot_asg_route;

    /// attack.vt_asg_land
    [value(97)]
    static uint32 slot_asg_route_done;

    /// the attack subgroups: attack_subgroup_stats, attack.vt_asg_land's attack_subgroup_land_stats
    [value(20)]
    static uint32 slot_asg_stats;

    /// the attack subgroups: update(now)
    [value(90)]
    static uint32 slot_asg_update;

    /// the selected group (player +0x9BC): unsigned thiscall(group) (EEGroup, both builds)
    [value(6)]
    static uint32 slot_group_count;

    /// the selected group: ee_unit *thiscall(group, i) (EEGroup, both builds: 16 slots, its vector +0x24)
    [value(5)]
    static uint32 slot_group_get;

    /// attack.vt_route_planner: route_planner_cost
    [value(1)]
    static uint32 slot_route_cost;

    [address((ee)0x00826610, (aoc)0x0083A6D8)]
    static vftable vt_asg_air;

    /// EECPASGLand
    [address((ee)0x00826CA8, (aoc)0x0083AD7C)]
    static vftable vt_asg_land;

    [address((ee)0x00826F10, (aoc)0x0083AFEC)]
    static vftable vt_asg_sea;

    [address((ee)0x008270F8, (aoc)0x0083B1D8)]
    static vftable vt_asg_sea_transport;

    /// EEAIGPGModifierLandAttackSubGroupRoutePlanner
    [address((ee)0x00825CC4, (aoc)0x00839C70)]
    static vftable vt_route_planner;
}

/// the game's functions attack.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class attack_functions
{
    [address((ee)0x00450554, (aoc)0x00451239), thiscall]
    [call_site("group_stat", (ee)0x00450546, (aoc)0x0045122B)]
    static extern void group_stat(void* stats, EEUnit* u);

    /// point from int x, y (the task location's point)
    [address((ee)0x0040D29D, (aoc)0x0044D28B), thiscall]
    static extern void point_set(void* point, int32* pt);

    [address((ee)0x0063A355, (aoc)0x006509AF), cdecl]
    [call_site("range_sort", (ee)0x0063052E, (aoc)0x0064631B)]
    static extern void range_sort(void** b, void** e, EEUnit* unit);

}
