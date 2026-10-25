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

    /// its own
    [offset((ee)0x190, (aoc)0x194), found(c)]
    EEPlayer* player;

    [offset((ee)0x1D0, (aoc)0x1D4), found(c)]
    int32 state;

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
    [offset(0x48), found(c)]
    int32 player_index;

    /// its slot 1: int thiscall(edge*, node*)
    [address((ee)0x004141E8, (aoc)0x00410074)]
    extern int32 cost(int32* edge, ee_route_node* node);
}

/// an attack strategy (EECPCombatStrategy)
[c_type("ee_strategy"), vtable((ee)0x0082795C, (aoc)0x0083C0A4)]
unsafe struct EECPCombatStrategy
{
    [offset(0x3C), found(s)]
    int32 _0x3c;

    [offset(0x40), found(s)]
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

    [offset(0x20), found(c)]
    EEPlayer* target_player;

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
