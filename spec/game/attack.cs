/// an attack group (EECPAttackGroup)
[c_type("ee_attack_group"), c_prefix("ag")]
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
[c_type("ee_combat_mgr"), c_prefix("am")]
unsafe struct EECPCombatManager
{
    [offset(0x48), found(c)]
    ee_attack_cmd* commander;

    [offset(0x4C), found(b)]
    uint8* _0x4c;

    [offset(0x62), found(v)]
    uint8 att_62;
}

/// the object at the combat manager's +0x4C
[c_type("void")]
unsafe struct ee_combat_mgr_4c
{
    [offset(0x48), found(s)]
    block _0x48;
}

/// the combat manager's commands, AM +0x48
[c_prefix("ac")]
unsafe struct ee_attack_cmd
{
    /// the groups' deque (its count +0x10)
    [offset(0x4), found(s)]
    block groups;
}

/// an attack subgroup (EECPAttackSubGroup, EECPASG*)
[c_type("ee_attack_subgroup"), c_prefix("asg")]
unsafe struct EECPAttackSubGroup
{
    /// its members
    [offset(0x54), found(c)]
    EECPAttackSubGroupUnit** units_begin;

    [offset(0x60), found(c)]
    block route;

    [offset(0x70), found(c)]
    uint32 waypoints;

    [offset(0x100), found(c)]
    ee_asg_target target_rec;

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
}

/// an attack subgroup's unit record (EECPAttackSubGroupUnit, EECPASGU*)
[c_type("ee_asg_unit"), c_prefix("asgu")]
unsafe struct EECPAttackSubGroupUnit
{
    [offset(0x44), found(c)]
    EEUnit* unit;

    /// set while it fights
    [offset(0x48), found(c)]
    void* engaged;

    [offset(0x4C), found(c)]
    EECPAttackSubGroup* subgroup;
}

/// what FN_ASGU_SCORE scores
[c_prefix("score_info")]
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
[c_type("ee_route_planner"), c_prefix("planner")]
unsafe struct EEAIGPGModifierLandAttackSubGroupRoutePlanner
{
    [offset(0x48), found(c)]
    int32 player_index;
}

/// an attack strategy (EECPCombatStrategy)
[c_type("ee_strategy")]
unsafe struct EECPCombatStrategy
{
    [offset(0x3C), found(s)]
    int32 _0x3c;

    [offset(0x40), found(s)]
    uint8 _0x40;
}

/// an attack task (EECPAGTask)
[c_type("ee_task")]
unsafe struct EECPAGTask
{
    [offset(0x20), found(c)]
    EEPlayer* target_player;

    [offset(0x28), found(c)]
    int32 _0x28;

    [offset(0x2C), found(c)]
    int32 _0x2c;

    [offset(0x30), found(c)]
    uint8 _0x30;
}
