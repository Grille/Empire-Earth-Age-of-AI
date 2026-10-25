/// a unit kind's movement, the physics model (EEPhysicsModel; EEPMNaval, EEPMLand, EEPMHelicopter ... - AoC's vtables have a slot more before the speed and turn functions)
[c_type("ee_physics_model"), vtable((ee)0x0082D010, (aoc)0x00842848)]
unsafe struct EEPhysicsModel
{
    /// EEPhysicsModel::vf9 thiscall(model, unit, distance to the waypoint, angle off the bow): 2 hold, 3 faster, 4
    /// slower - more than 45 degrees off, slower
    [address((ee)0x00548645, (aoc)0x00554CB2)]
    extern int32 speed_decide(EEUnit* u, float32 dist, float32 angle);

    /// EEPhysicsModel::vf8 thiscall(model, unit, distance to the waypoint, angle off the bow): the new speed from
    /// model_speed_decide (given the same) - on the last waypoint faster no further than the distance
    [address((ee)0x0054858E, (aoc)0x00554BDE)]
    extern float32 speed_step(EEUnit* u, float32 dist, float32 angle);

    /// EEPhysicsModel slot 11 (AoC 12: its vtables have a slot more before; AoC rewrote it - the signs, a step ahead
    /// -, so not patched there) thiscall(model, unit, turn rate now (unit +0x17C, radians/s), angle left to turn): 3
    /// turn faster towards it, 4 slower - from a stopping angle of the type's +0xC8 x the settings' +0x88
    [address((ee)0x0054880E, (aoc)0x00554F56)]
    extern int32 turn_decide(EEUnit* u, float32 rate, float32 left);

    /// EEPhysicsModel::vf7 thiscall(model, unit, float pos[3], float target[3], out angle left, out turn rate, out
    /// new heading, 0 / 1 to the unit's facing): the tick's turn towards a point (EEPMNaval::vf1 0x54EB00 gives the
    /// front waypoint); ret 0x1C
    [address((ee)0x005484D9, (aoc)0x00554A25)]
    extern void turn_towards(EEUnit* u, float32* pos, float32* target, float32* left, float32* rate, float32* heading, char8 facing);

    /// vf18 (AoC 19): the collision step, thiscall(model, unit) - after each movement mode's step (0x549F82 +0x48),
    /// while the unit moves (+0x2D8, AoC +0x2DC) and has contacts (+0xC8) or a blocked time (+0x248): the contacts
    /// tested (slot 14, AoC 15: the speed cut, the push +0x230/+0x234) and the push applied (movement.slot_pm_push) -
    /// docs/engine-notes.md "Collision"
    [slot((ee)18, (aoc)19), address((ee)0x00549E61, (aoc)0x00556710)]
    extern void collide(EEUnit* u);
}

/// movement's data of the game, places in its code and numbers
static unsafe class movement
{
    /// EEPhysicsModel: the push applied (EEPhysicsModel.collide's +0xC; EEPMNaval::vf3 0x54F05E, AoC 0x55B9D1)
    [value(3)]
    static uint32 slot_pm_push;

    /// EEPhysicsModel: model_speed_decide (AoC: its vf8 0x554BDE calls +0x28)
    [value((ee)9, (aoc)10)]
    static uint32 slot_pm_speed_decide;

    /// EEPhysicsModel: model_speed_step (the tick's +0x20)
    [value((ee)8, (aoc)9)]
    static uint32 slot_pm_speed_step;

    /// EEPhysicsModel: model_turn_decide (called from the turn step 0x548266 / AoC 0x554738: +0x2C / +0x30)
    [value((ee)11, (aoc)12)]
    static uint32 slot_pm_turn_decide;

    /// EEPhysicsModel: turn_towards (the tick's +0x1C)
    [value(7)]
    static uint32 slot_pm_turn_towards;

    /// EEPMHovercraft
    [address((ee)0x0082CE8C, (aoc)0x008426B8)]
    static vftable vt_pm_hover;

    /// EEPMLand, the land units' movement
    [address((ee)0x0082CFB0, (aoc)0x008427E8)]
    static vftable vt_pm_land;

    /// EEPMNaval, the ships' movement
    [address((ee)0x0082CE30, (aoc)0x00842658)]
    static vftable vt_pm_naval;

    /// EEPMSubmergedNaval, the submarines' movement
    [address((ee)0x0082CDD4, (aoc)0x008425F8)]
    static vftable vt_pm_submerged;
}

/// the land units' movement (EEPMLand)
[@base(typeof(EEPhysicsModel)), c_type("ee_physics_model")]
unsafe struct EEPMLand
{
    /// its collision step: a thunk to EEPhysicsModel's (collide) - the same function in the vtables of EEPMNaval,
    /// EEPMSubmergedNaval, EEPMHovercraft and EEPMHelicopter (movement.vt_pm_*; AoC also EEPMSpaceship), which the
    /// mod's collision hooks replace (game/api/movement.c)
    [address((ee)0x0054CABF, (aoc)0x0055BE0E), overrides(typeof(EEPhysicsModel))]
    extern void collide(EEUnit* u);
}
