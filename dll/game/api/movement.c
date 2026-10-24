// THE MOVEMENT MODELS (EEPhysicsModel and its kinds: EEPMNaval, EEPMLand ...; one per kind of unit, shared): hooks.
// Each tick a model steers its unit towards the front waypoint (vtable +0x1C, the turn) and sets its speed (+0x20,
// EEPhysicsModel::vf8) from a decision (+0x24, FN_SPEED_DECIDE): 2 hold, 3 faster (up to FN_UNIT_MAX_SPEED; on the
// last waypoint no faster than the distance), 4 slower (by the acceleration a tick, 0 below 0.005), else stop. The
// decision: more than 45 degrees off the bow (+-pi/4: 0x82C5C0, 0x82B668), slower - the unit crawls at its least
// speed while it turns, then speeds up; within, faster; slower too when the way left is no longer than it needs to
// stop. docs/engine-notes.md "Routes".

// HOOKS
// the speed decision of the ships' and the submarines' models (their vtables' slot; the land units', aircraft's and
// the others' keep the game's) - the hook calls model_speed_decide for what it leaves to the game
typedef int (__thiscall *speed_decide_hook)(void *model, ee_unit *u, float dist, float angle);
internal inline int model_speed_decide(void *model, ee_unit *u, float dist, float angle) { return GF(FN_SPEED_DECIDE)(model, u, dist, angle); }
internal inline int patch_ship_speed_decide(speed_decide_hook h) {
    return patch_vtable(GA(VT_PM_NAVAL), GA(SLOT_PM_SPEED_DECIDE), GA(FN_SPEED_DECIDE), (void *)h)
         + patch_vtable(GA(VT_PM_SUBMERGED), GA(SLOT_PM_SPEED_DECIDE), GA(FN_SPEED_DECIDE), (void *)h);
}
// the turn decision (the turn step 0x548266 asks it each tick with the unit's turn rate now, +0x17C, and the angle
// left, both signed): 3 turn faster towards it (up to FN_UNIT_MAX_TURN), 4 slower (by the turn acceleration a tick, 0
// at the end), 2 hold. The 2017 update's takes both as sizes and doesn't look a step ahead, so a unit swings past the
// heading and back; AoC's (slot 12) has both - not patched there (0)
typedef int (__thiscall *turn_decide_hook)(void *model, ee_unit *u, float rate, float left);
internal inline int model_turn_decide(void *model, ee_unit *u, float rate, float left) { return GF(FN_TURN_DECIDE)(model, u, rate, left); }
internal inline int patch_ship_turn_decide(turn_decide_hook h) {
    if (build_is("aoc")) return 0;
    return patch_vtable(GA(VT_PM_NAVAL), GA(SLOT_PM_TURN_DECIDE), GA(FN_TURN_DECIDE), (void *)h)
         + patch_vtable(GA(VT_PM_SUBMERGED), GA(SLOT_PM_TURN_DECIDE), GA(FN_TURN_DECIDE), (void *)h);
}
